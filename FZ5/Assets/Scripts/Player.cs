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

    private Vector3 move;

    Rigidbody player;

    private PlayerInput PlayerInput;
    private InputAction _move;
    private InputAction _jump;
    private InputAction _dash;
    private InputAction _fire;
    private InputAction _block;

    void Start()
    {
        player = GetComponent<Rigidbody>();
        PlayerInput = GetComponent<PlayerInput>();

        dashTimer = -1f;
        dashingTime = -1f;

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
        #endregion
    }


    private void Update()
    {
        IsGrounded();

        UpdateDash();
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

    }

    public void Block()
    {

    }

    private void SetRotation()
    {
        rotation = Vector3.Angle(Vector3.right, move);
        if (move.y < 0)
            rotation *= -1;
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
        {
            isGrounded = true;
            isJumping = false;
        }

        return isGrounded;
    }
}