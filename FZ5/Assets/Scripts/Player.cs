using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.InputSystem;

public class Player : MonoBehaviour
{
    [Header("Movement")]
    [SerializeField] private float maxSpeed;
    [SerializeField] private float acceleration;
    [SerializeField] private float deceleration;

    [Header("Jump")]
    [SerializeField] private int jumpVelocity;
    [SerializeField] private float lowJumpMult;
    [SerializeField] private float fallMult;

    [Header("Dash")]
    [SerializeField] private float dashTime;
    [SerializeField] private float dashDistance;
    [SerializeField] private float dashCoolDown;
    [SerializeField] private float coefDecelDash;

    private bool canDash;
    private bool isJumping;
    private bool isDashing;
    private bool isGrounded;

    private float rotation;
    private float dashTimer;
    private float dashingTime;
    private float sensitivity;
    private float rotX;
    private float rotY;

    private Vector2 mousePos;
    private Vector2 mouseDelta;
    private Vector3 move;
    private Quaternion cam;

    private Rigidbody player;

    private PlayerInput PlayerInput;
    private InputAction _move;
    private InputAction _jump;
    private InputAction _dash;
    private InputAction _fire;
    private InputAction _block;
    private InputAction _mousePos;
    private InputAction _mouseDelta;

    void Start()
    {
        player = GetComponent<Rigidbody>();
        PlayerInput = GetComponent<PlayerInput>();

        dashTimer = -1f;
        dashingTime = -1f;
        sensitivity = 100f;

        rotX = player.transform.rotation.eulerAngles.x > 180 ? player.transform.rotation.eulerAngles.x - 360 : player.transform.rotation.eulerAngles.x;
        rotY = player.transform.rotation.eulerAngles.y > 180 ? player.transform.rotation.eulerAngles.y - 360 : player.transform.rotation.eulerAngles.y;
        rotX *= -1;

        #region Input Actions
        _move = PlayerInput.actions.FindAction("Move");
        _move.performed += ctx => move = ctx.ReadValue<Vector2>();
        _move.canceled += _ => move = Vector2.zero;

        _jump = PlayerInput.actions.FindAction("Jump");
        _jump.performed += _ => Jump();
        _jump.canceled += _ => isJumping = true;

        _dash = PlayerInput.actions.FindAction("Dash");
        _dash.performed += _ => Dash();

        _fire = PlayerInput.actions.FindAction("Fire");
        _fire.performed += _ => Fire();

        _block = PlayerInput.actions.FindAction("Block");
        _block.performed += _ => Block();

        _mousePos = PlayerInput.actions.FindAction("MousePos");
        _mousePos.performed += ctx => mousePos = ctx.ReadValue<Vector2>();

        _mouseDelta = PlayerInput.actions.FindAction("MouseDelta");
        _mouseDelta.performed += ctx => mouseDelta = ctx.ReadValue<Vector2>();
        #endregion
    }


    private void Update()
    {
        IsGrounded();

        UpdateDash();

        rotY += (mouseDelta.x * Time.deltaTime * sensitivity);
        player.transform.rotation = Quaternion.Euler(0f, rotY, 0f);
    }

    private void FixedUpdate()
    {
        Move();

        // Jump gravity
        if (player.velocity.y > 0)
            player.velocity += fallMult * Physics.gravity.y * Time.deltaTime * Vector3.up;
        else if (player.velocity.y < 0 || isJumping)
            player.velocity += lowJumpMult * Physics.gravity.y * Time.deltaTime * Vector3.up;
    }

    public void Jump()
    {
        if (isGrounded)
            player.velocity = Vector3.up * jumpVelocity;
    }

    public void Dash()
    {
        if (canDash && move != Vector3.zero)
        {
            canDash = false;
            isDashing = true;
            dashingTime = dashTime;
            dashTimer = dashCoolDown;

            SetVelocity(dashDistance / dashTime);
        }
    }
    private void UpdateDash()
    {
        if (dashTimer > 0)
            dashTimer -= Time.deltaTime;
        else
            canDash = true;

        if (dashingTime > 0)
            dashingTime -= Time.deltaTime;
        else
            isDashing = false;
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
        Ray ray = Camera.main.ScreenPointToRay(mousePos);

        if (Physics.Raycast(ray, out RaycastHit hit) && hit.collider.gameObject.CompareTag("Player"))
        {
            Destroy(hit.collider.gameObject);
        }
    }

    public void Block()
    {

    }

    private void SetRotation()
    {
        rotation = Vector3.Angle(player.transform.right, move);
        if (move.y < 0)
            rotation *= -1;
    }

    private void SetVelocity(float speed)
    {
        player.velocity = (Time.deltaTime * speed * move.x * player.transform.right
                        + Time.deltaTime * speed * player.velocity.y * player.transform.up
                        + Time.deltaTime * speed * move.y * player.transform.forward);
    }

    private void AddVelocity(float speed)
    {
        player.velocity += (Time.deltaTime * speed * move.x * player.transform.right
                         + Time.deltaTime * speed * player.velocity.y * player.transform.up
                         + Time.deltaTime * speed * move.y * player.transform.forward);
    }

    private bool IsGrounded()
    {
        LayerMask mask = (1 << 8);
        mask = ~mask;
        isGrounded = false;

        if (Physics.Raycast(transform.position + Vector3.right * (-0.2f), Vector3.down, 1.2f, mask) || Physics.Raycast(transform.position + Vector3.right * (0.2f), Vector3.down, 1.2f, mask) || Physics.Raycast(transform.position, Vector3.down, 1.2f, mask) || Physics.Raycast(transform.position + Vector3.forward * (-0.2f), Vector3.down, 1.2f, mask) || Physics.Raycast(transform.position + Vector3.forward * (0.2f), Vector3.down, 1.2f, mask))
        {
            isGrounded = true;
            isJumping = false;
        }

        return isGrounded;
    }
}