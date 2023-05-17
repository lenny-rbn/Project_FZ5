using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.InputSystem;

public class Player : MonoBehaviour
{
    [Header("Movement")]
    [SerializeField] private float maxSpeed;
    [SerializeField] private float acceleration;
    [SerializeField] private float deceleration; // not used

    [Header("Jump")]
    [SerializeField] private int jumpVelocity;
    [SerializeField] private float jumpWindow;
    [SerializeField] private float jumpGravity;

    [Header("Dash")]
    [SerializeField] private float dashTime;
    [SerializeField] private float dashDistance;
    [SerializeField] private float dashCoolDown;
    [SerializeField] private float coefDecelDash;

    [Header("Block")]
    [SerializeField] private float blockTime;
    [SerializeField] private float blockCoolDown;

    [Header("Attack")]
    [SerializeField] private float attackTime;
    [SerializeField] private float attackCoolDown;

    [Header("Camera")]
    [SerializeField] private float sensitivity;

    private bool canDash;
    private bool canBlock;
    private bool canAttack;
    private bool isMelee;
    private bool isDashing;
    private bool isGrounded;
    private bool isBlocking;
    private bool isAttacking;

    private float rotX;
    private float rotY;
    private float rotation;

    private float jumpBuffer;

    private float dashTimer;
    private float blockTimer;
    private float attackTimer;
    private float dashingTime;
    private float blockingTime;
    private float attackingTime;

    private Vector2 mousePos;
    private Vector2 mouseDelta;
    private Vector3 move;

    public GameObject weapon;

    private Rigidbody player;
    private new Renderer renderer;

    private PlayerInput PlayerInput;
    private InputAction _move;
    private InputAction _jump;
    private InputAction _dash;
    private InputAction _fire;
    private InputAction _block;
    private InputAction _switch;
    private InputAction _mousePos;
    private InputAction _mouseDelta;

    void Start()
    {
        player = GetComponent<Rigidbody>();
        PlayerInput = GetComponent<PlayerInput>();
        renderer = player.GetComponent<Renderer>();

        Cursor.lockState = CursorLockMode.Locked;
        Cursor.visible = false;

        isMelee = true;
        dashTimer = -1f;
        blockTimer = -1f;
        jumpBuffer = -1f;
        dashingTime = -1f;
        attackTimer = -1f;

        #region Input Actions
        _move = PlayerInput.actions.FindAction("Move");
        _move.performed += ctx => move = ctx.ReadValue<Vector2>();
        _move.canceled += _ => move = Vector2.zero;

        _jump = PlayerInput.actions.FindAction("Jump");
        _jump.performed += _ => Jump();

        _dash = PlayerInput.actions.FindAction("Dash");
        _dash.performed += _ => Dash();

        _fire = PlayerInput.actions.FindAction("Fire");
        _fire.performed += _ => Fire();

        _block = PlayerInput.actions.FindAction("Block");
        _block.performed += _ => Block();

        _switch = PlayerInput.actions.FindAction("Switch");
        _switch.performed += _ => Switch();

        _mousePos = PlayerInput.actions.FindAction("MousePos");
        _mousePos.performed += ctx => mousePos = ctx.ReadValue<Vector2>();

        _mouseDelta = PlayerInput.actions.FindAction("MouseDelta");
        _mouseDelta.performed += ctx => mouseDelta = ctx.ReadValue<Vector2>();
        #endregion
    }

    private void UpdateCamera()
    {
        rotY += (mouseDelta.x * Time.deltaTime * sensitivity);
        rotX += (mouseDelta.y * Time.deltaTime * sensitivity);
        rotX = Mathf.Clamp(rotX, -90f, 90f);

        player.transform.rotation = Quaternion.Euler(0f, rotY, 0f);
        Camera.main.transform.parent.rotation = Quaternion.Euler(-rotX, rotY, 0f);

        mouseDelta = Vector2.zero;
    }

    private void FixedUpdate()
    {
        Move();

        // Jump gravity
        if (player.velocity.y != 0)
            player.velocity += jumpGravity * Physics.gravity.y * Time.deltaTime * Vector3.up;
    }

    public void Jump()
    {
        jumpBuffer = jumpWindow;
    }

    public void Dash()
    {
        if (canDash && !isAttacking && !isBlocking && move != Vector3.zero)
        {
            canDash = false;
            isDashing = true;
            dashingTime = dashTime;
            dashTimer = dashCoolDown;

            if (player.velocity.y < 0f)
                player.velocity = new Vector3(Mathf.Cos(rotation * Mathf.Deg2Rad) * dashDistance / dashTime, 0f, Mathf.Sin(rotation * Mathf.Deg2Rad) * dashDistance / dashTime);
            else
                SetVelocity(dashDistance / dashTime);
        }
    }

    private void UpdateStates()
    {
        // Jump
        if (jumpBuffer > 0f)
            jumpBuffer -= Time.deltaTime;

        // Dash
        if (dashTimer > 0f)
            dashTimer -= Time.deltaTime;
        else
            canDash = true;

        if (dashingTime > 0f)
        {
            dashingTime -= Time.deltaTime;
            renderer.material.SetColor("_Color", Color.blue);
        }
        else
            isDashing = false;

        // Attack
        if (attackTimer > 0f)
            attackTimer -= Time.deltaTime;
        else
            canAttack = true;

        if (attackingTime > 0f)
        {
            attackingTime -= Time.deltaTime;
            renderer.material.SetColor("_Color", Color.red);
        }
        else
            isAttacking = false;

        // Block
        if (blockTimer > 0f)
            blockTimer -= Time.deltaTime;
        else
            canBlock = true;

        if (blockingTime > 0f)
        {
            blockingTime -= Time.deltaTime;
            renderer.material.SetColor("_Color", Color.green);
        }
        else
            isBlocking = false;

        if (!isDashing && !isAttacking && !isBlocking)
            renderer.material.SetColor("_Color", Color.clear);
    }

    public void Move()
    {
        if (move != Vector3.zero)
        {
            if (!isDashing)
            {
                SetRotation();
                AddVelocity(acceleration);
            }

            if (isDashing)
                SetVelocity(dashDistance / dashTime);
            else if (player.velocity.magnitude > maxSpeed)
                SetVelocity(maxSpeed);
        }
        else
        {
            if (isDashing)
                SetVelocity(dashDistance / dashTime);
            else
                player.velocity = new Vector3(0f, player.velocity.y, 0f);
        }
    }

    public void Fire()
    {
        if (canAttack)
        {
            // Canceling
            canBlock = true;

            canAttack = false;
            isAttacking = true;
            attackingTime = attackTime;
            attackTimer = attackCoolDown;

            Ray ray = Camera.main.ScreenPointToRay(mousePos);

            if (Physics.Raycast(ray, out RaycastHit hit) && hit.collider.gameObject.CompareTag("Player"))
                Destroy(hit.collider.gameObject);
        }
    }

    public void Block()
    {
        if (canBlock)
        {
            canBlock = false;
            isBlocking = true;
            blockingTime = blockTime;
            blockTimer = blockCoolDown;
        }
    }

    public void Switch()
    {
        canAttack = true;

        if (isMelee)
        {
            isMelee = false;
            weapon.transform.rotation = Quaternion.Euler(90f, 0f, 0f);
        }
        else
        {
            isMelee = true;
            weapon.transform.rotation = Quaternion.Euler(0f, 0f, 0f);
        }
    }

    private void SetRotation()
    {
        rotation = Vector3.Angle(Vector3.right, player.transform.forward);

        // Set rotation to 0-360Â° angles
        if (player.transform.rotation.eulerAngles.y > 90f && player.transform.rotation.eulerAngles.y < 270f)
            rotation = 360f - rotation;

        if (move.y == 0) rotation -= 90f * move.x;
        else rotation -= 45f * move.x * move.y;

        if (move.y < 0) rotation = rotation < 180f ? rotation + 180f : rotation - 180f;
    }

    private void SetVelocity(float speed)
    {
        player.velocity = new Vector3(Mathf.Cos(rotation * Mathf.Deg2Rad) * speed, player.velocity.y, Mathf.Sin(rotation * Mathf.Deg2Rad) * speed);
    }

    private void AddVelocity(float speed)
    {
        player.velocity += new Vector3(Mathf.Cos(rotation * Mathf.Deg2Rad) * speed, 0f, Mathf.Sin(rotation * Mathf.Deg2Rad) * speed);
    }

    private bool IsGrounded()
    {
        LayerMask mask = (1 << 8);
        mask = ~mask;
        isGrounded = false;

        if (Physics.Raycast(transform.position + Vector3.right * (-0.2f), Vector3.down, 1.2f, mask) || Physics.Raycast(transform.position + Vector3.right * (0.2f), Vector3.down, 1.2f, mask) || Physics.Raycast(transform.position, Vector3.down, 1.2f, mask) || Physics.Raycast(transform.position + Vector3.forward * (-0.2f), Vector3.down, 1.2f, mask) || Physics.Raycast(transform.position + Vector3.forward * (0.2f), Vector3.down, 1.2f, mask))
            isGrounded = true;

        return isGrounded;
    }

    private void Update()
    {
        IsGrounded();
        UpdateStates();

        if (jumpBuffer > 0f && isGrounded)
        {
            Debug.Log("Buffered time left: " + (int)(jumpBuffer * 1000) + "ms");
            player.velocity = new Vector3(player.velocity.x, jumpVelocity, player.velocity.z);
            jumpBuffer = 0f;
        }

        UpdateCamera();
    }
}